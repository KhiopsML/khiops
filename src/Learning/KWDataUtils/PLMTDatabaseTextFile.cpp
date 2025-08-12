// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMTDatabaseTextFile.h"

PLMTDatabaseTextFile::PLMTDatabaseTextFile()
{
	lTotalFileSize = 0;
	lTotalUsedFileSize = 0;
	nDatabasePreferredBufferSize = 0;
	nMainLocalTableNumber = 0;
	lOutputNecessaryDiskSpace = 0;
	lEmptyOpenNecessaryMemory = 0;
	lMinOpenNecessaryMemory = 0;
	lMaxOpenNecessaryMemory = 0;
	lEstimatedMaxSecondaryRecordNumber = 0;
	lEstimatedMinSingleInstanceMemoryLimit = 0;
	lEstimatedMaxSingleInstanceMemoryLimit = 0;
	nReadSizeMin = 0;
	nReadSizeMax = 0;
	bOpenOnDemandMode = false;

	// Changement de driver : on prend le driver de table parallele
	KWDataTableDriver* parallelDataTableDriver = new PLDataTableDriverTextFile;
	parallelDataTableDriver->CopyFrom(dataTableDriverCreator);
	delete dataTableDriverCreator;
	dataTableDriverCreator = parallelDataTableDriver;
}

PLMTDatabaseTextFile::~PLMTDatabaseTextFile()
{
	oaUsedMappingHeaderLineClasses.DeleteAll();
	oaIndexedMappingsDataItemLoadIndexes.DeleteAll();
	oaIndexedMappingsMainKeyIndexes.DeleteAll();
}

void PLMTDatabaseTextFile::Reset()
{
	ivUsedMappingFlags.SetSize(0);
	oaUsedMappingHeaderLineClasses.DeleteAll();
	oaIndexedMappingsDataItemLoadIndexes.DeleteAll();
	oaIndexedMappingsMainKeyIndexes.DeleteAll();
	lvFileSizes.SetSize(0);
	lTotalFileSize = 0;
	lTotalUsedFileSize = 0;
	lvInMemoryEstimatedFileObjectNumbers.SetSize(0);
	lvEstimatedUsedMemoryPerObject.SetSize(0);
	nMainLocalTableNumber = 0;
	CleanOpenInformation();
}

boolean PLMTDatabaseTextFile::ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
						     PLMTDatabaseTextFile* outputDatabaseTextFile)
{
	boolean bOk = true;
	boolean bDisplay = GetPreparationTraceMode();
	boolean bCurrentVerboseMode;
	KWMTDatabaseMapping* mapping;
	KWMTDatabaseMapping* outputMapping;
	PLDataTableDriverTextFile* driver;
	KWLoadIndexVector* livDataItemLoadIndexes;
	IntVector* ivMainKeyIndexes;
	KWClass* kwcDriverLogicalClass;
	KWClass* kwcHeaderLineClass;
	KWClass* kwcUsedHeaderLineClass;
	longint lDatabaseClassNecessaryMemory;
	int nReferenceBufferSize;
	longint lMaxUsedFileSize;
	int i;

	require(outputDatabaseTextFile == NULL or bRead);
	require(not IsOpenedForRead() and not IsOpenedForWrite());

	// Initialisation des resultats
	CleanOpenInformation();

	// Calcul du nombre de table principale locales
	nMainLocalTableNumber = ComputeMainLocalTableNumber();

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

	// Initialisation recursive du mapping a partir de la table principale pour avoir des driver initialises
	DMTMPhysicalTerminateMapping(GetMainMapping());
	if (bRead)
	{
		// En lecture, on utilise la classe physique
		DMTMPhysicalInitializeMapping(GetMainMapping(), kwcPhysicalClass, true);
	}
	else
	{
		// En ecriture, on utile la classe logique
		DMTMPhysicalInitializeMapping(GetMainMapping(), kwcClass, false);
	}

	// Nettoyage prealable
	oaIndexedMappingsDataItemLoadIndexes.DeleteAll();
	oaIndexedMappingsMainKeyIndexes.DeleteAll();
	ivUsedMappingFlags.SetSize(0);

	// Dimensionnement des resultats de calcul bufferises
	if (lvFileSizes.GetSize() == 0)
	{
		lvFileSizes.SetSize(GetMultiTableMappings()->GetSize());
		lvInMemoryEstimatedFileObjectNumbers.SetSize(GetMultiTableMappings()->GetSize());
		lvEstimatedUsedMemoryPerObject.SetSize(GetMultiTableMappings()->GetSize());
		oaUsedMappingHeaderLineClasses.SetSize(GetMultiTableMappings()->GetSize());
	}

	// Dimensionnement des resultats de calcul non bufferises
	oaIndexedMappingsDataItemLoadIndexes.SetSize(GetMultiTableMappings()->GetSize());
	oaIndexedMappingsMainKeyIndexes.SetSize(GetMultiTableMappings()->GetSize());
	ivUsedMappingFlags.SetSize(GetMultiTableMappings()->GetSize());

	// Calcul des index pour tous les mappings, et recopie des caracteristiques des drivers
	// dans leur version serialisee
	lTotalFileSize = 0;
	lTotalUsedFileSize = 0;
	for (i = 0; i < GetMultiTableMappings()->GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
		driver = cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver());

		// Memorisation du mapping si driver utilise
		if (driver != NULL)
		{
			assert(mapping->GetClassName() != "");
			ivUsedMappingFlags.SetAt(i, 1);
		}

		// Memorisation de la taille du fichier d'entree
		if (bRead)
		{
			if (lvFileSizes.GetAt(i) == 0)
				lvFileSizes.SetAt(i, PLRemoteFileService::GetFileSize(mapping->GetDataTableName()));
			lTotalFileSize += lvFileSizes.GetAt(i);
			if (driver != NULL and not IsReferencedClassMapping(mapping))
				lTotalUsedFileSize += lvFileSizes.GetAt(i);
		}

		// Memorisation de l'estimation du nombre d'objets par fichier
		if (lvInMemoryEstimatedFileObjectNumbers.GetAt(i) == 0 and driver != NULL)
			lvInMemoryEstimatedFileObjectNumbers.SetAt(
			    i, driver->GetInMemoryEstimatedObjectNumber(lvFileSizes.GetAt(i)));

		// Memorisation de la memoire utilisee par KWObject
		lvEstimatedUsedMemoryPerObject.SetAt(i, 0);
		if (driver != NULL)
			lvEstimatedUsedMemoryPerObject.SetAt(i, driver->GetEstimatedUsedMemoryPerObject());

		// Si driver present, on calcule et memorise le vecteur d'index des champs de la table en entree
		if (bRead and driver != NULL)
		{
			// Recherche de la classe logique correspondant a la classe du driver
			kwcDriverLogicalClass =
			    KWClassDomain::GetCurrentDomain()->LookupClass(driver->GetClass()->GetName());
			check(kwcDriverLogicalClass);

			// Creation d'une classe fictive basee sur l'analyse de la premiere ligne du fichier, dans le
			// cas d'une ligne d'entete
			kwcUsedHeaderLineClass = NULL;
			if (GetHeaderLineUsed())
			{
				// Creation si necessaire d'un classe memorisant la ligne d'entete
				kwcHeaderLineClass = cast(KWClass*, oaUsedMappingHeaderLineClasses.GetAt(i));
				if (kwcHeaderLineClass == NULL)
				{
					kwcHeaderLineClass = new KWClass;
					oaUsedMappingHeaderLineClasses.SetAt(i, kwcHeaderLineClass);
				}

				// Calcul bufferise pour minimiser les acces au fichier
				if (kwcHeaderLineClass->GetName() == "")
				{
					// On passe temporairement par un mode verbeux, car ce calcul bufferise ne sera
					// effectue qu'une seule fois
					bCurrentVerboseMode = driver->GetVerboseMode();
					driver->SetVerboseMode(true);

					// Calcul d'une classe representant le header a partir du fichier
					kwcHeaderLineClass->SetName(
					    kwcClass->GetDomain()->BuildClassName(GetClassName() + "HeaderLine"));
					bOk = cast(PLDataTableDriverTextFile*, driver)
						  ->BuildDataTableClass(kwcHeaderLineClass);
					assert(bOk or kwcHeaderLineClass->GetName() == "");

					// Restitution du mode initial
					driver->SetVerboseMode(bCurrentVerboseMode);
				}
				kwcUsedHeaderLineClass = kwcHeaderLineClass;
			}

			// Indexation du driver
			bOk = bOk and driver->ComputeDataItemLoadIndexes(kwcDriverLogicalClass, kwcUsedHeaderLineClass);
			if (not bOk)
				break;

			// Memorisation du vecteur des indexes des champs
			assert(oaIndexedMappingsDataItemLoadIndexes.GetAt(i) == NULL);
			livDataItemLoadIndexes = driver->GetDataItemLoadIndexes()->Clone();
			oaIndexedMappingsDataItemLoadIndexes.SetAt(i, livDataItemLoadIndexes);

			// Memorisation du vecteur des indexes des champs de la cle
			assert(oaIndexedMappingsMainKeyIndexes.GetAt(i) == NULL);
			ivMainKeyIndexes = driver->GetMainKeyIndexes()->Clone();
			oaIndexedMappingsMainKeyIndexes.SetAt(i, ivMainKeyIndexes);

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
						    driver->ComputeNecessaryDiskSpaceForFullWrite(kwcDriverLogicalClass,
												  lvFileSizes.GetAt(i));
					}
				}
			}
		}
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
	// De toutes facons, il faudra que la tache suive regulierement la place restante en ecriture
	if (GetSelectionAttribute() != "")
		lOutputNecessaryDiskSpace /= 20;

	// Calcul de la taille de buffer preferree max et de la taille de fichier max pour les fichiers utilises, hors
	// tables externes
	nDatabasePreferredBufferSize = 0;
	lMaxUsedFileSize = 0;
	for (i = 0; i < GetMainTableNumber(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
		driver = cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver());

		// En lecture, on ne prend que les fichiers multi-table utilises
		if (bRead and driver != NULL)
		{
			nDatabasePreferredBufferSize =
			    max(nDatabasePreferredBufferSize,
				PLRemoteFileService::GetPreferredBufferSize(mapping->GetDataTableName()));
			lMaxUsedFileSize = max(lMaxUsedFileSize, lvFileSizes.GetAt(i));
		}
		// En ecriture, on prend tous les fichiers multi-tables
		else if (not bRead)
			nDatabasePreferredBufferSize =
			    max(nDatabasePreferredBufferSize,
				PLRemoteFileService::GetPreferredBufferSize(mapping->GetDataTableName()));
	}

	// Exigences sur la taille du buffer de lecture
	nReadSizeMin = SystemFile::nMinPreferredBufferSize;
	nReadSizeMax =
	    (SystemFile::nMaxPreferredBufferSize / nDatabasePreferredBufferSize) * nDatabasePreferredBufferSize;
	nReadSizeMax = max(nReadSizeMax, nReadSizeMin);

	// En multi-table, on ne prevoie la presence d'un cache que pour la cas max
	// En effet, en cas de tres nombreuses tables secondaires, on pourrait manquer de memoire pour avor un cache
	// systematiquement
	if (bRead)
		nReadSizeMax += nDatabasePreferredBufferSize;

	// En mode lecture, on limite la taille des buffer par la taille des fichiers en entree
	if (bRead)
	{
		if (nReadSizeMin > lMaxUsedFileSize)
			nReadSizeMin = (int)lMaxUsedFileSize;
		if (nReadSizeMax > lMaxUsedFileSize)
			nReadSizeMax = (int)lMaxUsedFileSize;
	}

	// Calcul des min et max de la memoire necessaire en fonction de taille de buffer min et max
	if (bOk)
	{
		// Mise a jour des min et max de memoire necessaire, en tenant compte des tailles de fichier
		lMinOpenNecessaryMemory += ComputeBufferNecessaryMemory(bRead, nReadSizeMin);
		lMaxOpenNecessaryMemory += ComputeBufferNecessaryMemory(bRead, nReadSizeMax);
	}

	// Calcul des informations necessaires pour la DatabaseMemoryGuard
	if (bOk and bRead)
	{
		ComputeMemoryGuardOpenInformation();

		// Mise a jour des min et max de memoire necessaire, en tenant compte du DatabaseMemoryGuard
		lMinOpenNecessaryMemory += lEstimatedMinSingleInstanceMemoryLimit;
		lMaxOpenNecessaryMemory += lEstimatedMaxSingleInstanceMemoryLimit;
	}

	// Affichage
	if (bDisplay)
	{
		cout << "PLMTDatabaseTextFile::ComputeOpenInformation " << bRead << endl;
		cout << "\tDatabaseClassNecessaryMemory\t"
		     << LongintToHumanReadableString(lDatabaseClassNecessaryMemory) << endl;
		cout << "\tEmptyOpenNecessaryMemory\t" << LongintToHumanReadableString(lEmptyOpenNecessaryMemory)
		     << endl;
		cout << "\tMinOpenBufferSize\t" << LongintToHumanReadableString(nReadSizeMin) << endl;
		cout << "\tMinOpenNecessaryMemory\t" << LongintToHumanReadableString(lMinOpenNecessaryMemory) << endl;
		cout << "\tMaxOpenBufferSize\t" << LongintToHumanReadableString(nReadSizeMax) << endl;
		cout << "\tMaxOpenNecessaryMemory\t" << LongintToHumanReadableString(lMaxOpenNecessaryMemory) << endl;
		cout << "\tEstimatedMinSingleInstanceMemoryLimit\t"
		     << LongintToHumanReadableString(lEstimatedMinSingleInstanceMemoryLimit) << endl;
		cout << "\tEstimatedMaxSingleInstanceMemoryLimit\t"
		     << LongintToHumanReadableString(lEstimatedMaxSingleInstanceMemoryLimit) << endl;
	}

	// Nettoyage
	DMTMPhysicalTerminateMapping(GetMainMapping());
	if (bRead)
		DeletePhysicalClass();
	kwcClass = NULL;

	// Nettoyage des resultat si KO
	if (not bOk)
		CleanOpenInformation();

	ensure(not bOk or GetTableNumber() == ivUsedMappingFlags.GetSize());
	return bOk;
}

boolean PLMTDatabaseTextFile::IsOpenInformationComputed() const
{
	return lMinOpenNecessaryMemory > 0;
}

void PLMTDatabaseTextFile::CleanOpenInformation()
{
	nDatabasePreferredBufferSize = 0;
	lOutputNecessaryDiskSpace = 0;
	lEmptyOpenNecessaryMemory = 0;
	lMinOpenNecessaryMemory = 0;
	lMaxOpenNecessaryMemory = 0;
	nReadSizeMin = 0;
	nReadSizeMax = 0;
	oaIndexedMappingsDataItemLoadIndexes.DeleteAll();
	oaIndexedMappingsMainKeyIndexes.DeleteAll();
	lEstimatedMinSingleInstanceMemoryLimit = 0;
	lEstimatedMaxSingleInstanceMemoryLimit = 0;
}

KWMTDatabaseMapping* PLMTDatabaseTextFile::GetUsedMappingAt(int nTableIndex) const
{
	require(IsOpenInformationComputed());
	require(GetTableNumber() == ivUsedMappingFlags.GetSize());
	require(0 <= nTableIndex and nTableIndex < GetTableNumber());

	// On est sur d'etre toujours en phase avec les mappings de la base, meme s'ils
	// ont ete mis a jour par un appel a UpdateMultiTableMappings
	if (ivUsedMappingFlags.GetAt(nTableIndex) == 0)
		return NULL;
	else
		return mappingManager.GetMappingAt(nTableIndex);
}

int PLMTDatabaseTextFile::ComputeUsedMappingNumber() const
{
	int nUsedMappingNumber;
	int i;

	require(IsOpenInformationComputed());
	require(GetTableNumber() == ivUsedMappingFlags.GetSize());

	// Il suffit de compter le nombre d'elements non nuls, sans avoir besoin de resynchroniser le tableau
	nUsedMappingNumber = 0;
	for (i = 0; i < ivUsedMappingFlags.GetSize(); i++)
		nUsedMappingNumber += ivUsedMappingFlags.GetAt(i);
	return nUsedMappingNumber;
}

const KWClass* PLMTDatabaseTextFile::GetUsedMappingHeaderLineClassAt(int nIndex) const
{
	require(IsOpenInformationComputed());
	return cast(const KWClass*, oaUsedMappingHeaderLineClasses.GetAt(nIndex));
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

int PLMTDatabaseTextFile::GetDatabasePreferredBuferSize() const
{
	require(IsOpenInformationComputed());
	return nDatabasePreferredBufferSize;
}

const LongintVector* PLMTDatabaseTextFile::GetInMemoryEstimatedFileObjectNumbers() const
{
	require(IsOpenInformationComputed());
	return &lvInMemoryEstimatedFileObjectNumbers;
}

const LongintVector* PLMTDatabaseTextFile::GetEstimatedUsedMemoryPerObject() const
{
	require(IsOpenInformationComputed());
	return &lvEstimatedUsedMemoryPerObject;
}

int PLMTDatabaseTextFile::GetMainLocalTableNumber() const
{
	return nMainLocalTableNumber;
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

longint PLMTDatabaseTextFile::GetEstimatedMaxSecondaryRecordNumber() const
{
	require(IsOpenInformationComputed());
	return lEstimatedMaxSecondaryRecordNumber;
}

longint PLMTDatabaseTextFile::GetEstimatedMinSingleInstanceMemoryLimit() const
{
	require(IsOpenInformationComputed());
	return lEstimatedMinSingleInstanceMemoryLimit;
}

longint PLMTDatabaseTextFile::GetEstimatedMaxSingleInstanceMemoryLimit() const
{
	require(IsOpenInformationComputed());
	return lEstimatedMaxSingleInstanceMemoryLimit;
}

int PLMTDatabaseTextFile::GetMaxSlaveProcessNumber() const
{
	require(IsOpenInformationComputed());
	return (int)ceil((lTotalUsedFileSize + 1.0) / (nReadSizeMin + 1));
}

int PLMTDatabaseTextFile::ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory, int nProcessNumber) const
{
	longint lOpenNecessaryMemory;
	longint lOpenGrantedMemoryForBuffers;
	int nLowerBufferSize;
	int nUpperBufferSize;
	int nBufferSize;

	require(IsOpenInformationComputed());
	require(lMinOpenNecessaryMemory <= lOpenGrantedMemory);
	require(lOpenGrantedMemory <= lMaxOpenNecessaryMemory);
	require(nProcessNumber >= 1);

	// Cas ou on a alloue le min
	nBufferSize = 0;
	if (lOpenGrantedMemory == lMinOpenNecessaryMemory)
		nBufferSize = nReadSizeMin;
	// Cas ou on a alloue le min
	else if (lOpenGrantedMemory == lMaxOpenNecessaryMemory)
		nBufferSize = nReadSizeMax;
	// Cas intermediaire
	else
	{
		// Calcul de la partie de la memoire pour les buffers
		lOpenGrantedMemoryForBuffers =
		    lOpenGrantedMemory - ComputeEstimatedSingleInstanceMemoryLimit(lOpenGrantedMemory);

		// On recherche par dichotomie la taille de buffer permettant d'utiliser au mieux la memoire alouee
		nLowerBufferSize = nReadSizeMin;
		nUpperBufferSize = nReadSizeMax;
		while (nUpperBufferSize - nLowerBufferSize > MemSegmentByteSize)
		{
			nBufferSize = (nUpperBufferSize + nLowerBufferSize) / 2;

			// Calcul de la memoire necessaire avec la taille de buffer courante
			lOpenNecessaryMemory =
			    lEmptyOpenNecessaryMemory + ComputeBufferNecessaryMemory(bRead, nBufferSize);

			// Dichotomie
			if (lOpenNecessaryMemory < lOpenGrantedMemoryForBuffers)
				nLowerBufferSize = nBufferSize;
			else
				nUpperBufferSize = nBufferSize;
		}

		// En lecture, chaque esclave doit lire au moins 5 buffer (pour que le travail soit bien reparti entre
		// les esclaves)
		if (bRead)
		{
			if (lTotalUsedFileSize / (nProcessNumber * 5) < nBufferSize)
				nBufferSize = int(lTotalUsedFileSize / (nProcessNumber * 5));
			nBufferSize = InputBufferedFile::FitBufferSize(nBufferSize);
		}

		// Arrondi a un multiple de preferredBufferSize du buffer de lecture
		assert(nDatabasePreferredBufferSize > 0);
		if (nBufferSize > nDatabasePreferredBufferSize)
			nBufferSize = (nBufferSize / nDatabasePreferredBufferSize) * nDatabasePreferredBufferSize;

		// Projection sur les exigences min et max
		nBufferSize = max(nBufferSize, nReadSizeMin);
		nBufferSize = min(nBufferSize, nReadSizeMax);
		ensure(lEmptyOpenNecessaryMemory + ComputeBufferNecessaryMemory(bRead, nBufferSize) <=
		       lOpenGrantedMemoryForBuffers);
	}
	assert(SystemFile::nMinPreferredBufferSize <= nBufferSize or
	       lTotalUsedFileSize < SystemFile::nMinPreferredBufferSize);
	ensure(nBufferSize > 0 or lTotalUsedFileSize == 0);
	return nBufferSize;
}

longint PLMTDatabaseTextFile::ComputeEstimatedSingleInstanceMemoryLimit(longint lOpenGrantedMemory) const
{
	longint lEstimatedSingleInstanceMemoryLimit;
	double dPercentage;

	require(IsOpenInformationComputed());
	require(lMinOpenNecessaryMemory <= lOpenGrantedMemory);
	require(lOpenGrantedMemory <= lMaxOpenNecessaryMemory);

	// Cas ou on a alloue le min
	if (lOpenGrantedMemory == lMinOpenNecessaryMemory)
		lEstimatedSingleInstanceMemoryLimit = lEstimatedMinSingleInstanceMemoryLimit;
	// Cas ou on a alloue le min
	else if (lOpenGrantedMemory == lMaxOpenNecessaryMemory)
		lEstimatedSingleInstanceMemoryLimit = lEstimatedMaxSingleInstanceMemoryLimit;
	// Cas intermediaire
	else
	{
		// Calcul selon une regle de 3
		dPercentage = (lOpenGrantedMemory - lMinOpenNecessaryMemory) * 1.0 /
			      (lMaxOpenNecessaryMemory - lMinOpenNecessaryMemory);
		lEstimatedSingleInstanceMemoryLimit = lEstimatedMinSingleInstanceMemoryLimit +
						      longint(dPercentage * (lEstimatedMaxSingleInstanceMemoryLimit -
									     lEstimatedMinSingleInstanceMemoryLimit));
	}
	ensure(lEstimatedMinSingleInstanceMemoryLimit <= lEstimatedSingleInstanceMemoryLimit and
	       lEstimatedSingleInstanceMemoryLimit <= lEstimatedMaxSingleInstanceMemoryLimit);
	ensure(lEstimatedSingleInstanceMemoryLimit <= lOpenGrantedMemory);
	return lEstimatedSingleInstanceMemoryLimit;
}

void PLMTDatabaseTextFile::SetOpenOnDemandMode(boolean bValue)
{
	bOpenOnDemandMode = bValue;
}

boolean PLMTDatabaseTextFile::GetOpenOnDemandMode() const
{
	return bOpenOnDemandMode;
}

void PLMTDatabaseTextFile::SetBufferSize(int nSize)
{
	cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetBufferSize(nSize);
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

			// Gestion fine du buffer
			inputBuffer->SetBufferSize(GetBufferSize());
			if (GetOpenOnDemandMode() and FileService::IsLocalURI(mapping->GetDataTableName()))
				inputBuffer->SetOpenOnDemandMode(GetOpenOnDemandMode());
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
				break;
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

			// Gestion fine du buffer
			outputBuffer->SetBufferSize(GetBufferSize());
			if (GetOpenOnDemandMode() and FileService::IsLocalURI(mapping->GetDataTableName()))
				outputBuffer->SetOpenOnDemandMode(GetOpenOnDemandMode());
			outputBuffer->SetOpenOnDemandMode(GetOpenOnDemandMode());
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
		Global::ActivateErrorFlowControl();
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
						// Il faut fermer tous les buffers: donc pas de break
						AddError("Cannot close output file " + outputBuffer->GetFileName());
					}
				}
			}
		}
		Global::DesactivateErrorFlowControl();
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

void PLMTDatabaseTextFile::SetLastReadMainKey(const KWObjectKey* objectKey)
{
	KWMTDatabaseMapping* mainMapping;

	require(IsOpenedForRead());

	mainMapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(0));
	check(mainMapping);
	assert(mainMapping->GetLastReadKey()->GetSize() == 0);
	assert(mainMapping->GetLastReadObject() == NULL);
	mainMapping->SetLastReadKey(objectKey);
}

void PLMTDatabaseTextFile::CleanMapping(KWMTDatabaseMapping* mapping)
{
	PLDataTableDriverTextFile* driver;

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

	// Nettoyage de l'index de l'enregistrement
	driver = GetDriverAt(mapping);
	check(driver);
	driver->SetBeginPosition(0);
	driver->SetEndPosition(0);
	driver->SetRecordIndex(0);
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

void PLMTDatabaseTextFile::ComputeMemoryGuardOpenInformation()
{
	boolean bDisplay = false;
	int i;
	KWMTDatabaseMapping* mapping;
	longint lMainObjectNumber;
	longint lSecondaryRecordNumber;
	longint lUsedMemoryPerObject;
	double dAverageSecondaryRecordNumber;
	longint lMinSecondaryRecordNumber;
	longint lMaxSecondaryRecordNumber;
	longint lTotalSecondaryRecordNumber;
	double dTableRatio;
	longint lUsedMemoryForTableSelection;

	// Le dimensionnement des buffer doit etre effectue
	require(lMinOpenNecessaryMemory > 0);

	// Initialisation des resultats
	lEstimatedMaxSecondaryRecordNumber = 0;
	lEstimatedMinSingleInstanceMemoryLimit = 0;
	lEstimatedMaxSingleInstanceMemoryLimit = 0;
	if (bDisplay)
		cout << "ComputeMemoryGuardOpenInformation\n";

	// Nombre de records de la table principale
	assert(GetUsedMappingAt(0) != NULL);
	lMainObjectNumber = GetInMemoryEstimatedFileObjectNumbers()->GetAt(0);

	// Comptage du nombre total d'enregistrements secondaires
	lTotalSecondaryRecordNumber = 0;
	for (i = 1; i < GetMainTableNumber(); i++)
	{
		mapping = GetUsedMappingAt(i);
		if (mapping != NULL)
			lTotalSecondaryRecordNumber += GetInMemoryEstimatedFileObjectNumbers()->GetAt(i);
	}

	// Parcours des mapping des tables principales utilises pour prendre en compte les besoins pour la gestion des
	// instances elephants
	for (i = 0; i < GetMainTableNumber(); i++)
	{
		mapping = GetUsedMappingAt(i);
		assert(mapping == NULL or mapping == mappingManager.GetMappingAt(i));

		// Prise en compte si si mapping utilise
		if (mapping != NULL)
		{
			// Stats sur les records de la table secondaires
			lSecondaryRecordNumber = GetInMemoryEstimatedFileObjectNumbers()->GetAt(i);
			lUsedMemoryPerObject = GetEstimatedUsedMemoryPerObject()->GetAt(i);

			// Cas de la table principale
			if (i == 0)
			{
				lEstimatedMinSingleInstanceMemoryLimit = lUsedMemoryPerObject;
				lEstimatedMaxSingleInstanceMemoryLimit = lUsedMemoryPerObject;
				lMinSecondaryRecordNumber = 0;
				lMaxSecondaryRecordNumber = 0;
			}
			// Prise en compte des records des tables secondaires
			else
			{
				dAverageSecondaryRecordNumber = 1 + lSecondaryRecordNumber / (1.0 + lMainObjectNumber);

				// Estimation du nombre min de records a traiter, selon un facteur par rapport au nombre
				// moyen de record par instance principale
				lMinSecondaryRecordNumber =
				    longint(dAverageSecondaryRecordNumber *
					    KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberFactor());
				lMaxSecondaryRecordNumber =
				    longint(dAverageSecondaryRecordNumber *
					    KWDatabaseMemoryGuard::GetDefautMaxSecondaryRecordNumberFactor());

				// On prend egalement en compte des nombres absolus de records, repartis par table secondaire.
				// Pour le min, on prend le minimum, pour eviter de surestimer la memoire minimum necessaire.
				// En effet, on peut avoir une table principale petite, avec des tables secondaires
				// comportement une majorite d'enregistrements orphelins, ce qui fausse les estimations
				// et peut empecher a tort l'execution des taches
				dTableRatio = lSecondaryRecordNumber / (1.0 + lTotalSecondaryRecordNumber);
				lMinSecondaryRecordNumber =
				    min(lMinSecondaryRecordNumber,
					longint(dTableRatio *
						KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberLowerBound()));
				lMaxSecondaryRecordNumber =
				    max(lMaxSecondaryRecordNumber,
					longint(dTableRatio *
						KWDatabaseMemoryGuard::GetDefautMaxSecondaryRecordNumberUpperBound()));

				// On tronque si depassement du nombre de records secondaires
				lMinSecondaryRecordNumber = min(lMinSecondaryRecordNumber, lSecondaryRecordNumber);
				lMaxSecondaryRecordNumber = min(lMaxSecondaryRecordNumber, lSecondaryRecordNumber);

				// Dans le cas ou on sature sur le nombre total d'enregistrement, on differencie le min du max
				if (lMinSecondaryRecordNumber == lMaxSecondaryRecordNumber)
				{
					// Correction pour avoir un ratio "standard" entre les nombre min et max
					lMinSecondaryRecordNumber =
					    (lMaxSecondaryRecordNumber *
					     KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberFactor()) /
					    KWDatabaseMemoryGuard::GetDefautMaxSecondaryRecordNumberFactor();

					// Correction pour respecter le nombre minimum de records par instance
					lMinSecondaryRecordNumber = max(
					    lMinSecondaryRecordNumber,
					    longint(
						sqrt(KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberFactor()) *
						dAverageSecondaryRecordNumber));
					lMinSecondaryRecordNumber =
					    min(lMinSecondaryRecordNumber, lMaxSecondaryRecordNumber);
				}

				// Prise en compte pour le dimensionnement, en prenant un peu de marge pour les variable
				// secondaires memorisant des selections
				lUsedMemoryForTableSelection = sizeof(KWObject*) + sizeof(KWValueIndexPair) +
							       sizeof(ObjectArray*) + sizeof(ObjectArray);
				lEstimatedMaxSecondaryRecordNumber += lMinSecondaryRecordNumber;
				lEstimatedMinSingleInstanceMemoryLimit +=
				    lMinSecondaryRecordNumber * (lUsedMemoryPerObject + lUsedMemoryForTableSelection);
				lEstimatedMaxSingleInstanceMemoryLimit +=
				    lMaxSecondaryRecordNumber * (lUsedMemoryPerObject + lUsedMemoryForTableSelection);
			}

			// Affichage des details
			if (bDisplay)
			{
				if (i == 0)
					cout << "\t\tTable\tDataPath\tRecords\tRAM per record\tlEst. max records\tMin "
						"record\tMax record\tMin RAM\tMax RAM\n";
				cout << "\t\tTable" << i << "\t" << mapping->GetDataPath() << "\t"
				     << lSecondaryRecordNumber << "\t" << lUsedMemoryPerObject << "\t";
				cout << lEstimatedMaxSecondaryRecordNumber << "\t";
				cout << lMinSecondaryRecordNumber << "\t" << lMaxSecondaryRecordNumber << "\t";
				cout << lEstimatedMinSingleInstanceMemoryLimit << "\t"
				     << lEstimatedMaxSingleInstanceMemoryLimit << "\n";
			}
		}
	}

	// On utilise une limite minimale au nombre max de records secondaires, pour de pas declencher de warning
	// utilisateurs excessifs
	lEstimatedMaxSecondaryRecordNumber = max(lEstimatedMaxSecondaryRecordNumber,
						 KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberLowerBound());

	// On utilise une limite minimale a la memoire RAM, pour de pas declencher de warning utilisateurs excessifs
	lEstimatedMinSingleInstanceMemoryLimit += 2 * BufferedFile::nDefaultBufferSize;
	lEstimatedMaxSingleInstanceMemoryLimit += 2 * BufferedFile::nDefaultBufferSize;

	// Passage de la memoire logique a la memoire physique en RAM
	lEstimatedMinSingleInstanceMemoryLimit =
	    longint(lEstimatedMinSingleInstanceMemoryLimit * (1 + MemGetAllocatorOverhead()));
	lEstimatedMaxSingleInstanceMemoryLimit =
	    longint(lEstimatedMaxSingleInstanceMemoryLimit * (1 + MemGetAllocatorOverhead()));

	// Affichage
	if (bDisplay)
	{
		cout << "\tMaxSecondaryRecordNumber\t" << lEstimatedMaxSecondaryRecordNumber << endl;
		cout << "\tMinSingleInstanceMemoryLimit\t" << lEstimatedMinSingleInstanceMemoryLimit << endl;
		cout << "\tMaxSingleInstanceMemoryLimit\t" << lEstimatedMaxSingleInstanceMemoryLimit << endl;
	}
}

longint PLMTDatabaseTextFile::ComputeBufferNecessaryMemory(boolean bRead, int nBufferSize) const
{
	longint lNecessaryMemory;
	int i;
	KWMTDatabaseMapping* mapping;

	require(nBufferSize >= 0);

	// Parcours des mapping utilises pour prendre en compte les tailles des buffers
	lNecessaryMemory = 0;
	for (i = 0; i < GetTableNumber(); i++)
	{
		mapping = GetUsedMappingAt(i);
		assert(mapping == NULL or mapping == mappingManager.GetMappingAt(i));

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
	IntVector* ivMainKeyIndexes;
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
		ivMainKeyIndexes = NULL;
		assert(oaIndexedMappingsDataItemLoadIndexes.GetSize() == 0 or
		       oaIndexedMappingsDataItemLoadIndexes.GetSize() == mappingManager.GetMappingNumber());
		for (i = 0; i < oaIndexedMappingsDataItemLoadIndexes.GetSize(); i++)
		{
			usedMapping = mappingManager.GetMappingAt(i);
			if (usedMapping == mapping)
			{
				livDataItemLoadIndexes =
				    cast(KWLoadIndexVector*, oaIndexedMappingsDataItemLoadIndexes.GetAt(i));
				ivMainKeyIndexes = cast(IntVector*, oaIndexedMappingsMainKeyIndexes.GetAt(i));
				break;
			}
		}

		// Creation du driver parallele
		dataTableDriver = cast(PLDataTableDriverTextFile*, dataTableDriverCreator->Clone());
		if (livDataItemLoadIndexes != NULL)
		{
			// Initialisation des index des attributs, et des attribut de cle
			dataTableDriver->GetDataItemLoadIndexes()->CopyFrom(livDataItemLoadIndexes);
			dataTableDriver->GetMainKeyIndexes()->CopyFrom(ivMainKeyIndexes);

			// Calcul du nombre de champs de la cle, puis initilisation du driver pour cette taille de cle
			nLastReadKeySize = 0;
			for (i = 0; i < ivMainKeyIndexes->GetSize(); i++)
			{
				if (ivMainKeyIndexes->GetAt(i) >= 0)
					nLastReadKeySize++;
			}
			dataTableDriver->InitializeLastReadKeySize(nLastReadKeySize);
		}
		return dataTableDriver;
	}
}

int PLMTDatabaseTextFile::ComputeMainLocalTableNumber()
{
	int nNumber;
	KWMTDatabaseMapping* mapping;
	int nTable;

	// Comptage des tables principale avec mapping vers un fichier local
	nNumber = 0;
	for (nTable = 0; nTable < GetMainTableNumber(); nTable++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(nTable));
		if (mapping != NULL and mapping->GetClassName() != "" and
		    FileService::IsLocalURI(mapping->GetDataTableName()))
			nNumber++;
	}
	return nNumber;
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
	debug(boolean bInitialSilentMode);
	PLShared_ObjectArray shared_oaIndexedMappingsDataItemLoadIndexes(new PLShared_LoadIndexVector);
	PLShared_ObjectArray shared_oaIndexedMappingsMainKeyIndexes(new PLShared_IntVector);

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	// Acces a la base
	database = cast(PLMTDatabaseTextFile*, o);

	// Verification en mode silencieux pour eviter le message d'un des CheckPartially
	debug(bInitialSilentMode = database->GetSilentMode());
	debug(database->SetSilentMode(true));
	debug(assert(database->CheckPartially(false) or database->CheckPartially(true)));
	debug(database->SetSilentMode(bInitialSilentMode));
	assert(database->mappingManager.GetMappingNumber() == database->ivUsedMappingFlags.GetSize());

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

	// Ecriture des parametres specifiques au driver
	serializer->PutBoolean(database->GetOpenOnDemandMode());
	serializer->PutInt(database->GetBufferSize());

	// Ecriture des mappings (pour permettre leur reconstruction)
	serializer->PutInt(database->GetMultiTableMappings()->GetSize());
	for (i = 0; i < database->GetMultiTableMappings()->GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, database->GetMultiTableMappings()->GetAt(i));

		// Serialisation des informations de mapping
		serializer->PutBoolean(mapping->GetExternalTable());
		serializer->PutString(mapping->GetOriginClassName());
		serializer->PutString(mapping->GetClassName());
		serializer->PutStringVector(mapping->GetAttributeNames());
		serializer->PutString(mapping->GetDataTableName());
	}

	// Ecriture du vecteur d'indicateur des mappings utilises
	serializer->PutIntVector(&database->ivUsedMappingFlags);

	// Ecriture des informations d'ouverture de la base
	serializer->PutLongintVector(&database->lvFileSizes);
	serializer->PutLongint(database->lTotalFileSize);
	serializer->PutLongint(database->lTotalUsedFileSize);
	serializer->PutInt(database->nDatabasePreferredBufferSize);
	serializer->PutLongintVector(&database->lvInMemoryEstimatedFileObjectNumbers);
	serializer->PutLongintVector(&database->lvEstimatedUsedMemoryPerObject);
	serializer->PutInt(database->nMainLocalTableNumber);
	serializer->PutLongint(database->lOutputNecessaryDiskSpace);
	serializer->PutLongint(database->lEmptyOpenNecessaryMemory);
	serializer->PutLongint(database->lMinOpenNecessaryMemory);
	serializer->PutLongint(database->lMaxOpenNecessaryMemory);
	serializer->PutLongint(database->lEstimatedMaxSecondaryRecordNumber);
	serializer->PutLongint(database->lEstimatedMinSingleInstanceMemoryLimit);
	serializer->PutLongint(database->lEstimatedMaxSingleInstanceMemoryLimit);
	serializer->PutInt(database->nReadSizeMin);
	serializer->PutInt(database->nReadSizeMax);

	// Ecriture des index des attributs par mapping
	assert(database->oaIndexedMappingsDataItemLoadIndexes.GetSize() == 0 or
	       database->oaIndexedMappingsDataItemLoadIndexes.GetSize() ==
		   database->GetMultiTableMappings()->GetSize());
	shared_oaIndexedMappingsDataItemLoadIndexes.SerializeObject(serializer,
								    &database->oaIndexedMappingsDataItemLoadIndexes);

	// Ecriture des index des champs de la cle par mapping
	assert(database->oaIndexedMappingsMainKeyIndexes.GetSize() == 0 or
	       database->oaIndexedMappingsMainKeyIndexes.GetSize() == database->GetMultiTableMappings()->GetSize());
	shared_oaIndexedMappingsMainKeyIndexes.SerializeObject(serializer, &database->oaIndexedMappingsMainKeyIndexes);

	// Ecriture des parametres du memory guard
	serializer->PutLongint(database->GetMemoryGuardMaxSecondaryRecordNumber());
	serializer->PutLongint(database->GetMemoryGuardMaxCreatedRecordNumber());
	serializer->PutLongint(database->GetMemoryGuardSingleInstanceMemoryLimit());
	serializer->PutLongint(KWDatabaseMemoryGuard::GetCrashTestMaxSecondaryRecordNumber());
	serializer->PutLongint(KWDatabaseMemoryGuard::GetCrashTestMaxCreatedRecordNumber());
	serializer->PutLongint(KWDatabaseMemoryGuard::GetCrashTestSingleInstanceMemoryLimit());
}

void PLShared_MTDatabaseTextFile::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	PLMTDatabaseTextFile* database;
	int nMappingNumber;
	int i;
	KWMTDatabaseMapping* mapping;
	PLShared_ObjectArray shared_oaIndexedMappingsDataItemLoadIndexes(new PLShared_LoadIndexVector);
	PLShared_ObjectArray shared_oaIndexedMappingsMainKeyIndexes(new PLShared_IntVector);

	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	database = cast(PLMTDatabaseTextFile*, o);

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

	// Lecture des parametres specifiques au driver
	database->SetOpenOnDemandMode(serializer->GetBoolean());
	database->SetBufferSize(serializer->GetInt());

	// Lecture des mappings serialises
	nMappingNumber = serializer->GetInt();
	assert(database->mappingManager.GetMappingNumber() == 1);
	for (i = 0; i < nMappingNumber; i++)
	{
		// Le premier mapping est pre-existant (table principale)
		if (i == 0)
			mapping = database->GetMainMapping();
		// Les autre sont a creer
		else
		{
			mapping = new KWMTDatabaseMapping;
			database->mappingManager.GetMappings()->Add(mapping);
		}

		// Deserialisation du mapping
		mapping->SetExternalTable(serializer->GetBoolean());
		mapping->SetOriginClassName(serializer->GetString());
		mapping->SetClassName(serializer->GetString());
		serializer->GetStringVector(&mapping->svAttributeNames);
		mapping->SetDataTableName(serializer->GetString());
	}

	// Lecture du vecteur d'indicateur des mappings utilises
	serializer->GetIntVector(&database->ivUsedMappingFlags);
	assert(database->mappingManager.GetMappingNumber() == database->ivUsedMappingFlags.GetSize());

	// Lecture des informations d'ouverture de la base
	serializer->GetLongintVector(&database->lvFileSizes);
	database->lTotalFileSize = serializer->GetLongint();
	database->lTotalUsedFileSize = serializer->GetLongint();
	database->nDatabasePreferredBufferSize = serializer->GetInt();
	serializer->GetLongintVector(&database->lvInMemoryEstimatedFileObjectNumbers);
	serializer->GetLongintVector(&database->lvEstimatedUsedMemoryPerObject);
	database->nMainLocalTableNumber = serializer->GetInt();
	database->lOutputNecessaryDiskSpace = serializer->GetLongint();
	database->lEmptyOpenNecessaryMemory = serializer->GetLongint();
	database->lMinOpenNecessaryMemory = serializer->GetLongint();
	database->lMaxOpenNecessaryMemory = serializer->GetLongint();
	database->lEstimatedMaxSecondaryRecordNumber = serializer->GetLongint();
	database->lEstimatedMinSingleInstanceMemoryLimit = serializer->GetLongint();
	database->lEstimatedMaxSingleInstanceMemoryLimit = serializer->GetLongint();
	database->nReadSizeMin = serializer->GetInt();
	database->nReadSizeMax = serializer->GetInt();

	// Lecture des index des attributs par mapping
	shared_oaIndexedMappingsDataItemLoadIndexes.DeserializeObject(serializer,
								      &database->oaIndexedMappingsDataItemLoadIndexes);

	// Lecture des index des attributs de cle par mapping
	shared_oaIndexedMappingsMainKeyIndexes.DeserializeObject(serializer,
								 &database->oaIndexedMappingsMainKeyIndexes);

	// Lecture des parametres du memory guard
	database->SetMemoryGuardMaxSecondaryRecordNumber(serializer->GetLongint());
	database->SetMemoryGuardMaxCreatedRecordNumber(serializer->GetLongint());
	database->SetMemoryGuardSingleInstanceMemoryLimit(serializer->GetLongint());
	KWDatabaseMemoryGuard::SetCrashTestMaxSecondaryRecordNumber(serializer->GetLongint());
	KWDatabaseMemoryGuard::SetCrashTestMaxCreatedRecordNumber(serializer->GetLongint());
	KWDatabaseMemoryGuard::SetCrashTestSingleInstanceMemoryLimit(serializer->GetLongint());
}

inline Object* PLShared_MTDatabaseTextFile::Create() const
{
	return new PLMTDatabaseTextFile;
}
